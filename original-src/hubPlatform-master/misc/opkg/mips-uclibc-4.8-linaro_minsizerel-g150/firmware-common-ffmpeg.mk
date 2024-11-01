define Package/firmware-common-ffmpeg
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-ffmpeg
	VERSION=1.0
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += zlib (=1.2.8-1),
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libpthread
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
