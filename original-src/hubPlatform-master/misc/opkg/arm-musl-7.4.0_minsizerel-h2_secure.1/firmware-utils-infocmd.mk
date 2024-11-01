define Package/firmware-utils-infocmd
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-infocmd
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-bus
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-infocmd/description
	Just firmware-utils-infocmd package
endef

define Package/firmware-utils-infocmd/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-infocmd/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-infocmd))
