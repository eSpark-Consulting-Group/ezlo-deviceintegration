define Package/firmware-common-git_version
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-git_version
	VERSION=1.0
endef

define Package/firmware-common-git_version/description
	Just firmware-common-git_version package
endef

define Package/firmware-common-git_version/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-git_version/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-git_version))
