define Package/firmware-common-zipgateway
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-zipgateway
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-jsonc (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	DEPENDS += +firmware-common-jsonc
	DEPENDS += +libc
endef

define Package/firmware-common-zipgateway/description
	Just firmware-common-zipgateway package
endef

define Package/firmware-common-zipgateway/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-zipgateway/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-zipgateway))
