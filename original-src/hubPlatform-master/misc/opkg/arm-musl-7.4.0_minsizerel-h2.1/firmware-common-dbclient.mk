define Package/firmware-common-dbclient
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-dbclient
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-json_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	EXTRA_DEPENDS += openldap-LMDB (=0.9.22-1),
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-json_utils
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
	DEPENDS += +openldap-LMDB
endef

define Package/firmware-common-dbclient/description
	Just firmware-common-dbclient package
endef

define Package/firmware-common-dbclient/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-dbclient/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-dbclient))
