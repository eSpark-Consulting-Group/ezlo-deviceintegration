define Package/firmware-common-data_utils
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-data_utils
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-data_utils/description
	Just firmware-common-data_utils package
endef

define Package/firmware-common-data_utils/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-data_utils/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-data_utils))
