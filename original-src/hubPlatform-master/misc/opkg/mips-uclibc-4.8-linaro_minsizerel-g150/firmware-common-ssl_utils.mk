define Package/firmware-common-ssl_utils
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-ssl_utils
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +libc
	DEPENDS += +libopenssl
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-ssl_utils/description
	Just firmware-common-ssl_utils package
endef

define Package/firmware-common-ssl_utils/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-ssl_utils/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-ssl_utils))
