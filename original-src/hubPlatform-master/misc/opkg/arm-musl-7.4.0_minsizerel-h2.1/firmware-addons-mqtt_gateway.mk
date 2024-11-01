define Package/firmware-addons-mqtt_gateway
	$(call Package/hub-platform/Default)
	TITLE=firmware-addons-mqtt_gateway
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-addons-mqtt_gateway/description
	Just firmware-addons-mqtt_gateway package
endef

define Package/firmware-addons-mqtt_gateway/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-addons-mqtt_gateway/* $(1)/
endef

$(eval $(call BuildPackage,firmware-addons-mqtt_gateway))
