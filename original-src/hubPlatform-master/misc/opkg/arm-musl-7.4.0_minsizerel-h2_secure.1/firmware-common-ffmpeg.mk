define Package/firmware-common-ffmpeg
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-ffmpeg
	VERSION=1.0
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += zlib (=1.2.8-1),
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +zlib
endef

define Package/firmware-common-ffmpeg/description
	Just firmware-common-ffmpeg package
endef

define Package/firmware-common-ffmpeg/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-ffmpeg/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-ffmpeg))
