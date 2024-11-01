define Package/firmware-common-libubus
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-libubus
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-jsonc (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	DEPENDS += +firmware-common-jsonc
	DEPENDS += +libc
	DEPENDS += +libgcc
endef

define Package/firmware-common-libubus/description
	Just firmware-common-libubus package
endef

define Package/firmware-common-libubus/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-libubus/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-libubus))
