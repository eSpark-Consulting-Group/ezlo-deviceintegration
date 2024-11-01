define Package/firmware-core-notifications
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-notifications
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-daemon_module (=1.0),
	EXTRA_DEPENDS += firmware-common-data_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-smtp_client (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-daemon_module
	DEPENDS += +firmware-common-data_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-smtp_client
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-notifications/description
	Just firmware-core-notifications package
endef

define Package/firmware-core-notifications/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-notifications/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-notifications))
