define Package/firmware-dev-camera_test
	$(call Package/hub-platform/Default)
	TITLE=firmware-dev-camera_test
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-dev-camera_test/description
	Just firmware-dev-camera_test package
endef

define Package/firmware-dev-camera_test/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-dev-camera_test/* $(1)/
endef

$(eval $(call BuildPackage,firmware-dev-camera_test))
