local cloud_devices = assert(require "cloud_devices", "Fail to load cloud devices functionality")
cloud_devices.unsubscribe("HUB:zigbee/scripts/events_handling")
