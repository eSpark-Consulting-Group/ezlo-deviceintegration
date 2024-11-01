define Package/firmware-common-mosquitto
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-mosquitto
	VERSION=1.0
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	DEPENDS += +libc
	DEPENDS += +libgcc
endef

define Package/firmware-common-mosquitto/description
	Just firmware-common-mosquitto package
endef

define Package/firmware-common-mosquitto/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-mosquitto/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-mosquitto))
