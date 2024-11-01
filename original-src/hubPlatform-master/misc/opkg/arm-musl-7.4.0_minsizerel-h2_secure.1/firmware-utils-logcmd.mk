define Package/firmware-utils-logcmd
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-logcmd
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-cloud_storage (=1.0),
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-hub_settings (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-cloud_storage
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-hub_settings
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-logcmd/description
	Just firmware-utils-logcmd package
endef

define Package/firmware-utils-logcmd/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-logcmd/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-logcmd))
