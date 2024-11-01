define Package/firmware-addons-cameras
	$(call Package/hub-platform/Default)
	TITLE=firmware-addons-cameras
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libatomic (=4.8-linaro-1),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libcurl (=7.58.0-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libliveMedia (=2018.10.17-1),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libatomic
	DEPENDS += +libc
	DEPENDS += +libcurl
	DEPENDS += +libgcc
	DEPENDS += +libliveMedia
	DEPENDS += +liblua
	DEPENDS += +libopenssl
	DEPENDS += +libpthread
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-addons-cameras/description
	Just firmware-addons-cameras package
endef

define Package/firmware-addons-cameras/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-addons-cameras/* $(1)/
endef

$(eval $(call BuildPackage,firmware-addons-cameras))
