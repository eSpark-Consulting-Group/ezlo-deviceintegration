define Package/firmware-common-service_metrics_data
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-service_metrics_data
	VERSION=1.0
endef

define Package/firmware-common-service_metrics_data/description
	Just firmware-common-service_metrics_data package
endef

define Package/firmware-common-service_metrics_data/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-service_metrics_data/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-service_metrics_data))
