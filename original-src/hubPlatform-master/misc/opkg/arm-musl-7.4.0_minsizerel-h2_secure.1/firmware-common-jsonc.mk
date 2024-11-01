define Package/firmware-common-jsonc
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-jsonc
	VERSION=1.0
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	DEPENDS += +libc
	DEPENDS += +libgcc
endef

define Package/firmware-common-jsonc/description
	Just firmware-common-jsonc package
endef

define Package/firmware-common-jsonc/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-jsonc/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-jsonc))
