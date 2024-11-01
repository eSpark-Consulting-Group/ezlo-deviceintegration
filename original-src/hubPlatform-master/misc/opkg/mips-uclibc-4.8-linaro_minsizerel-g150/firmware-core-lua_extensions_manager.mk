define Package/firmware-core-lua_extensions_manager
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-lua_extensions_manager
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-lua_binding_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-extensions_manager_interface (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-lua_binding_interface
	DEPENDS += +firmware-core-extensions_manager_interface
	DEPENDS += +libc
	DEPENDS += +liblua
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-lua_extensions_manager/description
	Just firmware-core-lua_extensions_manager package
endef

define Package/firmware-core-lua_extensions_manager/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-lua_extensions_manager/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-lua_extensions_manager))
