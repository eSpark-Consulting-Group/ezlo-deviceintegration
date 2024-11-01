define Package/firmware-core-cored_model
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-cored_model
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-data_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-hub_settings (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-data_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-hub_settings
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-cored_model/description
	Just firmware-core-cored_model package
endef

define Package/firmware-core-cored_model/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-cored_model/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-cored_model))
