define Package/firmware-dev-camera_test
	$(call Package/hub-platform/Default)
	TITLE=firmware-dev-camera_test
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libc
	DEPENDS += +libgcc
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
