define Package/firmware-common-device_firmware_update_manager
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-device_firmware_update_manager
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-http (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-http
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-device_firmware_update_manager/description
	Just firmware-common-device_firmware_update_manager package
endef

define Package/firmware-common-device_firmware_update_manager/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-device_firmware_update_manager/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-device_firmware_update_manager))
