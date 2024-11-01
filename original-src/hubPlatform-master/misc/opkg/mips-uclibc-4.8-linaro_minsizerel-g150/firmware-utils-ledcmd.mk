define Package/firmware-utils-ledcmd
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-ledcmd
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-hardware_manager (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-bus
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-hardware_manager
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-ledcmd/description
	Just firmware-utils-ledcmd package
endef

define Package/firmware-utils-ledcmd/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-ledcmd/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-ledcmd))
