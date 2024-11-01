define Package/firmware-addons-zigbee
	$(call Package/hub-platform/Default)
	TITLE=firmware-addons-zigbee
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +liblua
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-addons-zigbee/description
	Just firmware-addons-zigbee package
endef

define Package/firmware-addons-zigbee/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-addons-zigbee/* $(1)/
endef

$(eval $(call BuildPackage,firmware-addons-zigbee))
