define Package/firmware-common-http
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-http
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libcurl (=7.58.0-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libcurl
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-http/description
	Just firmware-common-http package
endef

define Package/firmware-common-http/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-http/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-http))
