define Package/firmware-addons-lua
	$(call Package/hub-platform/Default)
	TITLE=firmware-addons-lua
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libcurl (=7.58.0-1),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libc
	DEPENDS += +libcurl
	DEPENDS += +libgcc
	DEPENDS += +liblua
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-addons-lua/description
	Just firmware-addons-lua package
endef

define Package/firmware-addons-lua/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-addons-lua/* $(1)/
endef

$(eval $(call BuildPackage,firmware-addons-lua))
