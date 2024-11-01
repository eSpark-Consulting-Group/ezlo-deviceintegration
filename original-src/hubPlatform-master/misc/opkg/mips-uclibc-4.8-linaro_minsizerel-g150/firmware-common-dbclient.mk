define Package/firmware-common-dbclient
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-dbclient
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-json_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	EXTRA_DEPENDS += openldap-LMDB (=0.9.22-1),
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-json_utils
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +librt
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
