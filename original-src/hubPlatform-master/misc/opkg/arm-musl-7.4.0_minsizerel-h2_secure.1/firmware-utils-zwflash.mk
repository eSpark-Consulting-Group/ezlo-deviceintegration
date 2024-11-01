define Package/firmware-utils-zwflash
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-zwflash
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-zwflash/description
	Just firmware-utils-zwflash package
endef

define Package/firmware-utils-zwflash/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-zwflash/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-zwflash))
