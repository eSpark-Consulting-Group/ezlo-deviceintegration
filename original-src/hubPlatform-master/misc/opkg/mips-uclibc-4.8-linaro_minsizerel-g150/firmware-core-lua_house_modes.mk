define Package/firmware-core-lua_house_modes
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-lua_house_modes
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-common_binding_interface (=1.0),
	EXTRA_DEPENDS += firmware-common-data_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-lua_binding_interface (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_house_modes (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_interface (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-common_binding_interface
	DEPENDS += +firmware-common-data_utils
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-lua_binding_interface
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-cored_house_modes
	DEPENDS += +firmware-core-cored_interface
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +liblua
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-lua_house_modes/description
	Just firmware-core-lua_house_modes package
endef

define Package/firmware-core-lua_house_modes/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-lua_house_modes/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-lua_house_modes))
