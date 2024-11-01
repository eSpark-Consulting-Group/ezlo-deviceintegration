define Package/firmware-common-config_reader
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-config_reader
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-config_reader/description
	Just firmware-common-config_reader package
endef

define Package/firmware-common-config_reader/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-config_reader/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-config_reader))
