define Package/firmware-common-json_utils
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-json_utils
	VERSION=1.0
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-json_utils/description
	Just firmware-common-json_utils package
endef

define Package/firmware-common-json_utils/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-json_utils/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-json_utils))
