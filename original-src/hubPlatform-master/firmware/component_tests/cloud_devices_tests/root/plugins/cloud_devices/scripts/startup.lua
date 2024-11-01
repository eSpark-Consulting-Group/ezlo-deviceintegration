local cloud_devices = assert(require "cloud_devices", "Fail to load cloud devices functionality")
cloud_devices.subscribe("HUB:cloud_devices/scripts/events_handling")
