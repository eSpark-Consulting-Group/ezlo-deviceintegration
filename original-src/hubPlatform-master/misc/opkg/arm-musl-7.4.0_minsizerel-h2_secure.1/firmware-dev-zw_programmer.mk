define Package/firmware-dev-zw_programmer
	$(call Package/hub-platform/Default)
	TITLE=firmware-dev-zw_programmer
	VERSION=1.0
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-dev-zw_programmer/description
	Just firmware-dev-zw_programmer package
endef

define Package/firmware-dev-zw_programmer/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-dev-zw_programmer/* $(1)/
endef

$(eval $(call BuildPackage,firmware-dev-zw_programmer))
