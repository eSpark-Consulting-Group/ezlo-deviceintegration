define Package/firmware-common-system_utils
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-system_utils
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-system_utils/description
	Just firmware-common-system_utils package
endef

define Package/firmware-common-system_utils/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-system_utils/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-system_utils))
