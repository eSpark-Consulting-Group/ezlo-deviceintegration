define Package/firmware-common-smtp_client
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-smtp_client
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libcurl (=7.58.0-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libcurl
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-smtp_client/description
	Just firmware-common-smtp_client package
endef

define Package/firmware-common-smtp_client/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-smtp_client/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-smtp_client))
