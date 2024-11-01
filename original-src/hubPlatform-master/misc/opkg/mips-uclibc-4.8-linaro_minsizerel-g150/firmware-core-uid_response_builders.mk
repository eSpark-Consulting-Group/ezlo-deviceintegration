define Package/firmware-core-uid_response_builders
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-uid_response_builders
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-data_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_house_modes (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_model (=1.0),
	EXTRA_DEPENDS += firmware-core-extensions_manager_interface (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-data_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-cored_house_modes
	DEPENDS += +firmware-core-cored_interface
	DEPENDS += +firmware-core-cored_model
	DEPENDS += +firmware-core-extensions_manager_interface
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-uid_response_builders/description
	Just firmware-core-uid_response_builders package
endef

define Package/firmware-core-uid_response_builders/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-uid_response_builders/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-uid_response_builders))
