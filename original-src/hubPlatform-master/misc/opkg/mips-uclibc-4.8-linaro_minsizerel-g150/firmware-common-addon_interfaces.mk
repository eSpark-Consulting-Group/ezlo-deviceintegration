define Package/firmware-common-addon_interfaces
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-addon_interfaces
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +libc
	DEPENDS += +libpthread
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
