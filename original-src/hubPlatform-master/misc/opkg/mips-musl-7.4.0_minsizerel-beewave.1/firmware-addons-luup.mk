define Package/firmware-addons-luup
	$(call Package/hub-platform/Default)
	TITLE=firmware-addons-luup
	VERSION=1.0
	EXTRA_DEPENDS += hub-platform-firmware (=2.0.69),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libmongoose (=2.8-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	EXTRA_DEPENDS += libupnp (=1.6.19-1),
	DEPENDS += +hub-platform-firmware
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libmongoose
	DEPENDS += +libstdcpp
	DEPENDS += +libupnp
endef

define Package/firmware-addons-luup/description
	Just firmware-addons-luup package
endef

define Package/firmware-addons-luup/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-addons-luup/* $(1)/
endef

$(eval $(call BuildPackage,firmware-addons-luup))
