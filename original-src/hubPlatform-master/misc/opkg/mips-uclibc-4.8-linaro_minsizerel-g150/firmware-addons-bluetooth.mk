define Package/firmware-addons-bluetooth
	$(call Package/hub-platform/Default)
	TITLE=firmware-addons-bluetooth
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += glib2 (=2.46.2-1),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libdbus (=1.9.14-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +glib2
	DEPENDS += +libc
	DEPENDS += +libdbus
	DEPENDS += +libgcc
	DEPENDS += +liblua
	DEPENDS += +libpthread
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-addons-bluetooth/description
	Just firmware-addons-bluetooth package
endef

define Package/firmware-addons-bluetooth/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-addons-bluetooth/* $(1)/
endef

$(eval $(call BuildPackage,firmware-addons-bluetooth))
