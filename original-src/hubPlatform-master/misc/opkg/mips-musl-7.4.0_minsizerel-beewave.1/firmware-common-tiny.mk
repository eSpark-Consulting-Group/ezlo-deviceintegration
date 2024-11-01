define Package/firmware-common-tiny
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-tiny
	VERSION=1.0
	EXTRA_DEPENDS += libatomic (=7.4.0-2),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	EXTRA_DEPENDS += libuv (=1.20.2_modified-1),
	EXTRA_DEPENDS += libwebsockets-custom (=3.1.0-1),
	DEPENDS += +libatomic
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
	DEPENDS += +libuv
	DEPENDS += +libwebsockets-custom
endef

define Package/firmware-common-tiny/description
	Just firmware-common-tiny package
endef

define Package/firmware-common-tiny/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-tiny/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-tiny))
