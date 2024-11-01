define Package/firmware-dev-dbreader
	$(call Package/hub-platform/Default)
	TITLE=firmware-dev-dbreader
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

define Package/firmware-dev-dbreader/description
	Just firmware-dev-dbreader package
endef

define Package/firmware-dev-dbreader/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-dev-dbreader/* $(1)/
endef

$(eval $(call BuildPackage,firmware-dev-dbreader))
