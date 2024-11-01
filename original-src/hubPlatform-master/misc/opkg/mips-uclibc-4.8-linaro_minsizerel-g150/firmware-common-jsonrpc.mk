define Package/firmware-common-jsonrpc
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-jsonrpc
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-jsonrpc/description
	Just firmware-common-jsonrpc package
endef

define Package/firmware-common-jsonrpc/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-jsonrpc/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-jsonrpc))
