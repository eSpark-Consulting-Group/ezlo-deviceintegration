define Package/firmware-common-addon_interfaces
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-addon_interfaces
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-addon_interfaces/description
	Just firmware-common-addon_interfaces package
endef

define Package/firmware-common-addon_interfaces/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-addon_interfaces/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-addon_interfaces))
