define Package/firmware-addons-zwave
	$(call Package/hub-platform/Default)
	TITLE=firmware-addons-zwave
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +liblua
	DEPENDS += +libopenssl
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-addons-zwave/description
	Just firmware-addons-zwave package
endef

define Package/firmware-addons-zwave/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-addons-zwave/* $(1)/
endef

$(eval $(call BuildPackage,firmware-addons-zwave))
