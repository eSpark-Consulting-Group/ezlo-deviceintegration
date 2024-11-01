define Package/firmware-common-ember_znet
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-ember_znet
	VERSION=1.0
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	DEPENDS += +libc
	DEPENDS += +libgcc
endef

define Package/firmware-common-ember_znet/description
	Just firmware-common-ember_znet package
endef

define Package/firmware-common-ember_znet/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-ember_znet/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-ember_znet))
