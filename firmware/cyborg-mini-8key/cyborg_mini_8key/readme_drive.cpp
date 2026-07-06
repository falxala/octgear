#include "readme_drive.h"

#include <string.h>

#include "Adafruit_TinyUSB.h"
#include "config.h"
#include "rescue_cmd_asset.h"

namespace {

constexpr uint16_t BLOCK_SIZE = 512;
constexpr uint8_t SECTORS_PER_CLUSTER = 1;
constexpr uint32_t FAT1_LBA = 1;
constexpr uint32_t FAT2_LBA = 2;
constexpr uint32_t ROOT_LBA = 3;
constexpr uint32_t DATA_LBA = 5;

constexpr char README_TEXT[] =
  "\xef\xbb\xbf"
  "Cyborg Mini 8 Keys\r\n"
  "\r\n"
  "このドライブはレスキューモードで起動したときだけ表示されます。\r\n"
  "レスキューモード中は、本体LEDが弱い緑色で点灯します。\r\n"
  "\r\n"
  "オンライン設定ページ:\r\n"
  "https://falxala.github.io/cyborg_mini_6keys/\r\n"
  "\r\n"
  "このドライブのファイル:\r\n"
  "- REMAPPER.URL: オンライン設定ページを開きます。\r\n"
  "- RESCUE.CMD: Windows用のオフライン救済プロンプトを開きます。\r\n"
  "\r\n"
  "通常はオンライン設定ページを使ってください。\r\n"
  "Web設定が使えない場合だけ RESCUE.CMD を実行します。\r\n"
  "救済プロンプトでは help でコマンド一覧、exit で終了できます。\r\n"
  "\r\n"
  "このドライブを表示するには、Key 5を押したままUSBに接続します。\r\n"
  "レスキューモードを終了するには、USBを抜いて通常どおり接続し直します。\r\n";

constexpr char README_EN_TEXT[] =
  "\xef\xbb\xbf"
  "Cyborg Mini 8 Keys\r\n"
  "\r\n"
  "This drive appears only when the device starts in rescue mode.\r\n"
  "The onboard LED stays dim green while rescue mode is active.\r\n"
  "\r\n"
  "Online key remapper:\r\n"
  "https://falxala.github.io/cyborg_mini_6keys/\r\n"
  "\r\n"
  "Files on this drive:\r\n"
  "- REMAPPER.URL opens the online remapper.\r\n"
  "- RESCUE.CMD opens a Windows offline rescue prompt.\r\n"
  "\r\n"
  "Use the online remapper for normal setup.\r\n"
  "Run RESCUE.CMD only when the web remapper is not available.\r\n"
  "At the rescue prompt, type help to see commands, or exit to close it.\r\n"
  "\r\n"
  "To show this drive, hold Key 5 while plugging in USB.\r\n"
  "To leave rescue mode, unplug USB and reconnect normally.\r\n";

constexpr char URL_TEXT[] =
  "[InternetShortcut]\r\n"
  "URL=https://falxala.github.io/cyborg_mini_6keys/\r\n";

constexpr uint32_t CLUSTER_SIZE = BLOCK_SIZE * SECTORS_PER_CLUSTER;
constexpr uint16_t clusterCount(uint32_t size) {
  return static_cast<uint16_t>((size + CLUSTER_SIZE - 1) / CLUSTER_SIZE);
}

constexpr uint16_t README_CLUSTER = 2;
constexpr uint16_t README_CLUSTERS = clusterCount(sizeof(README_TEXT) - 1);
constexpr uint16_t README_EN_CLUSTER = README_CLUSTER + README_CLUSTERS;
constexpr uint16_t README_EN_CLUSTERS = clusterCount(sizeof(README_EN_TEXT) - 1);
constexpr uint16_t URL_CLUSTER = README_EN_CLUSTER + README_EN_CLUSTERS;
constexpr uint16_t URL_CLUSTERS = clusterCount(sizeof(URL_TEXT) - 1);
constexpr uint16_t RESCUE_CMD_CLUSTER = URL_CLUSTER + URL_CLUSTERS;
constexpr uint16_t RESCUE_CMD_CLUSTERS = clusterCount(RescueCmdAsset::RESCUE_CMD_TEXT_SIZE);
constexpr uint16_t TOTAL_DATA_CLUSTERS =
  README_CLUSTERS + README_EN_CLUSTERS + URL_CLUSTERS + RESCUE_CMD_CLUSTERS;
constexpr uint32_t BLOCK_COUNT = DATA_LBA + (TOTAL_DATA_CLUSTERS * SECTORS_PER_CLUSTER);

struct DriveFile {
  const char* name;
  const uint8_t* data;
  uint32_t size;
  uint16_t firstCluster;
  uint16_t clusters;
};

const DriveFile DRIVE_FILES[] = {
  {"README  TXT", reinterpret_cast<const uint8_t*>(README_TEXT), sizeof(README_TEXT) - 1, README_CLUSTER, README_CLUSTERS},
  {"READMEENTXT", reinterpret_cast<const uint8_t*>(README_EN_TEXT), sizeof(README_EN_TEXT) - 1, README_EN_CLUSTER, README_EN_CLUSTERS},
  {"REMAPPERURL", reinterpret_cast<const uint8_t*>(URL_TEXT), sizeof(URL_TEXT) - 1, URL_CLUSTER, URL_CLUSTERS},
  {"RESCUE  CMD", RescueCmdAsset::RESCUE_CMD_TEXT, RescueCmdAsset::RESCUE_CMD_TEXT_SIZE, RESCUE_CMD_CLUSTER, RESCUE_CMD_CLUSTERS},
};

Adafruit_USBD_MSC readmeMsc;
bool driveActive = false;

bool readmeDriveEnabledAtBoot() {
  if (Config::README_DRIVE_ENABLED) {
    return true;
  }

  if (Config::README_DRIVE_ENABLE_KEY_INDEX >= Config::KEY_COUNT) {
    return false;
  }

  const uint8_t pin = Config::KEY_PINS[Config::README_DRIVE_ENABLE_KEY_INDEX];
  return digitalRead(pin) == LOW;
}

void putLe16(uint8_t* buffer, uint16_t offset, uint16_t value) {
  buffer[offset] = static_cast<uint8_t>(value & 0xff);
  buffer[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
}

void putLe32(uint8_t* buffer, uint16_t offset, uint32_t value) {
  buffer[offset] = static_cast<uint8_t>(value & 0xff);
  buffer[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
  buffer[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xff);
  buffer[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xff);
}

void putFat12Entry(uint8_t* fat, uint16_t cluster, uint16_t value) {
  const uint16_t offset = cluster + (cluster / 2);

  if ((cluster & 1) == 0) {
    fat[offset] = static_cast<uint8_t>(value & 0xff);
    fat[offset + 1] = static_cast<uint8_t>((fat[offset + 1] & 0xf0) | ((value >> 8) & 0x0f));
  } else {
    fat[offset] = static_cast<uint8_t>((fat[offset] & 0x0f) | ((value << 4) & 0xf0));
    fat[offset + 1] = static_cast<uint8_t>((value >> 4) & 0xff);
  }
}

void writeDirectoryEntry(
  uint8_t* buffer,
  uint16_t offset,
  const char name[11],
  uint8_t attributes,
  uint16_t cluster,
  uint32_t size
) {
  memcpy(buffer + offset, name, 11);
  buffer[offset + 11] = attributes;
  putLe16(buffer, offset + 26, cluster);
  putLe32(buffer, offset + 28, size);
}

void buildBootSector(uint8_t* buffer) {
  buffer[0] = 0xeb;
  buffer[1] = 0x3c;
  buffer[2] = 0x90;
  memcpy(buffer + 3, "MSDOS5.0", 8);
  putLe16(buffer, 11, BLOCK_SIZE);
  buffer[13] = SECTORS_PER_CLUSTER;
  putLe16(buffer, 14, 1); // reserved sectors
  buffer[16] = 2; // FAT count
  putLe16(buffer, 17, 32); // root entries
  putLe16(buffer, 19, BLOCK_COUNT);
  buffer[21] = 0xf8;
  putLe16(buffer, 22, 1); // sectors per FAT
  putLe16(buffer, 24, 1); // sectors per track
  putLe16(buffer, 26, 1); // heads
  buffer[36] = 0x80;
  buffer[38] = 0x29;
  putLe32(buffer, 39, 0x43384d42);
  memcpy(buffer + 43, "CYBORG8    ", 11);
  memcpy(buffer + 54, "FAT12   ", 8);
  buffer[510] = 0x55;
  buffer[511] = 0xaa;
}

void buildFatSector(uint8_t* buffer) {
  buffer[0] = 0xf8;
  buffer[1] = 0xff;
  buffer[2] = 0xff;

  for (const DriveFile& file : DRIVE_FILES) {
    for (uint16_t index = 0; index < file.clusters; index++) {
      const uint16_t cluster = file.firstCluster + index;
      const uint16_t value = index + 1 < file.clusters ? cluster + 1 : 0xfff;
      putFat12Entry(buffer, cluster, value);
    }
  }
}

void buildRootSector(uint8_t* buffer) {
  writeDirectoryEntry(buffer, 0, "CYBORG8    ", 0x08, 0, 0);

  for (uint8_t index = 0; index < sizeof(DRIVE_FILES) / sizeof(DRIVE_FILES[0]); index++) {
    const DriveFile& file = DRIVE_FILES[index];
    writeDirectoryEntry(buffer, 32 * (index + 1), file.name, 0x01, file.firstCluster, file.size);
  }
}

void readDataSector(uint32_t sector, uint8_t* block) {
  if (sector < DATA_LBA) {
    return;
  }

  const uint32_t dataSector = sector - DATA_LBA;
  const uint16_t cluster = static_cast<uint16_t>(2 + (dataSector / SECTORS_PER_CLUSTER));
  const uint8_t sectorInCluster = static_cast<uint8_t>(dataSector % SECTORS_PER_CLUSTER);

  for (const DriveFile& file : DRIVE_FILES) {
    if (cluster < file.firstCluster || cluster >= file.firstCluster + file.clusters) {
      continue;
    }

    const uint32_t fileOffset =
      ((cluster - file.firstCluster) * CLUSTER_SIZE) + (static_cast<uint32_t>(sectorInCluster) * BLOCK_SIZE);
    if (fileOffset >= file.size) {
      return;
    }

    const uint32_t remaining = file.size - fileOffset;
    const uint32_t length = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;
    memcpy(block, file.data + fileOffset, length);
    return;
  }
}

int32_t readCallback(uint32_t lba, void* buffer, uint32_t bufsize) {
  if (bufsize == 0 || (bufsize % BLOCK_SIZE) != 0) {
    return -1;
  }

  uint8_t* output = static_cast<uint8_t*>(buffer);

  for (uint32_t offset = 0; offset < bufsize; offset += BLOCK_SIZE) {
    const uint32_t sector = lba + (offset / BLOCK_SIZE);
    uint8_t* block = output + offset;
    memset(block, 0, BLOCK_SIZE);

    if (sector == 0) {
      buildBootSector(block);
    } else if (sector == FAT1_LBA || sector == FAT2_LBA) {
      buildFatSector(block);
    } else if (sector == ROOT_LBA) {
      buildRootSector(block);
    } else {
      readDataSector(sector, block);
    }
  }

  return static_cast<int32_t>(bufsize);
}

int32_t writeCallback(uint32_t, uint8_t*, uint32_t) {
  return -1;
}

void flushCallback() {
}

bool writableCallback() {
  return false;
}

}  // namespace

void beginReadmeDrive() {
  if (!readmeDriveRequestedAtBoot()) {
    return;
  }

  driveActive = true;
  readmeMsc.setID("Cyborg", "Remapper", "1.0");
  readmeMsc.setCapacity(BLOCK_COUNT, BLOCK_SIZE);
  readmeMsc.setReadWriteCallback(readCallback, writeCallback, flushCallback);
  readmeMsc.setWritableCallback(writableCallback);
  readmeMsc.setUnitReady(true);
  readmeMsc.begin();
}

bool readmeDriveRequestedAtBoot() {
  return readmeDriveEnabledAtBoot();
}

bool readmeDriveActive() {
  return driveActive;
}
