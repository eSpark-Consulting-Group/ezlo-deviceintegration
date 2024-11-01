define Package/firmware-dev-two_way_audio_test
	$(call Package/hub-platform/Default)
	TITLE=firmware-dev-two_way_audio_test
	VERSION=1.0
endef

define Package/firmware-dev-two_way_audio_test/description
	Just firmware-dev-two_way_audio_test package
endef

define Package/firmware-dev-two_way_audio_test/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-dev-two_way_audio_test/* $(1)/
endef

$(eval $(call BuildPackage,firmware-dev-two_way_audio_test))
