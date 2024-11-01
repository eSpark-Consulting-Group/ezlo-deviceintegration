define Package/firmware-dev-dbreader
	$(call Package/hub-platform/Default)
	TITLE=firmware-dev-dbreader
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

define Package/firmware-dev-dbreader/description
	Just firmware-dev-dbreader package
endef

define Package/firmware-dev-dbreader/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-dev-dbreader/* $(1)/
endef

$(eval $(call BuildPackage,firmware-dev-dbreader))
