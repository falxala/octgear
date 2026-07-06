import { useEffect, useRef } from "react";

import { sendRemapperHeartbeat } from "../../features/device/deviceCommands";
import type { WebHidTransport } from "../../features/device/webHidTransport";
import { t } from "../../shared/i18n";

const DEVICE_HEARTBEAT_INTERVAL_MS = 300;
const DEVICE_HEARTBEAT_TIMEOUT_MS = 700;

async function sendHeartbeatWithTimeout(transport: WebHidTransport) {
  let timeout = 0;

  try {
    await Promise.race([
      sendRemapperHeartbeat(transport),
      new Promise<never>((_, reject) => {
        timeout = window.setTimeout(() => reject(new Error(t.device.timeout)), DEVICE_HEARTBEAT_TIMEOUT_MS);
      }),
    ]);
  } finally {
    window.clearTimeout(timeout);
  }
}

export function useDeviceSession(
  transport: WebHidTransport,
  connected: boolean,
  onDisconnected: () => void,
) {
  const onDisconnectedRef = useRef(onDisconnected);
  const disconnectHandledRef = useRef(false);

  useEffect(() => {
    onDisconnectedRef.current = onDisconnected;
  });

  useEffect(() => {
    if (connected) {
      disconnectHandledRef.current = false;
    }
  }, [connected]);

  useEffect(() => {
    if (!connected) {
      return;
    }

    let cancelled = false;
    let timeout = 0;
    const handleDisconnected = async () => {
      if (disconnectHandledRef.current) {
        return;
      }
      disconnectHandledRef.current = true;
      cancelled = true;
      window.clearTimeout(timeout);
      await transport.close().catch(() => undefined);
      onDisconnectedRef.current();
    };
    const ping = async () => {
      try {
        await sendHeartbeatWithTimeout(transport);
      } catch {
        if (!cancelled) {
          await handleDisconnected();
        }
        return;
      }

      if (!cancelled) {
        timeout = window.setTimeout(() => void ping(), DEVICE_HEARTBEAT_INTERVAL_MS);
      }
    };
    void ping();

    return () => {
      cancelled = true;
      window.clearTimeout(timeout);
    };
  }, [connected, transport]);

  useEffect(() => {
    return transport.addDisconnectListener(async () => {
      if (disconnectHandledRef.current) {
        return;
      }
      disconnectHandledRef.current = true;
      await transport.close().catch(() => undefined);
      onDisconnectedRef.current();
    });
  }, [transport]);

  return async () => {
    disconnectHandledRef.current = true;
    await transport.close().catch(() => undefined);
    onDisconnectedRef.current();
  };
}
