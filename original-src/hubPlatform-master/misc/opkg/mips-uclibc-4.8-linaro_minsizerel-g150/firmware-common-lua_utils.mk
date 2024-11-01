define Package/firmware-common-lua_utils
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-lua_utils
	VERSION=1.0
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	DEPENDS += +libc
	DEPENDS += +liblua
	DEPENDS += +libpthread
endef

define Package/firmware-common-lua_utils/description
	Just firmware-common-lua_utils package
endef

define Package/firmware-common-lua_utils/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-lua_utils/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-lua_utils))
