// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/chromeos/network/network_list_detailed_view_base.h"

#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_delegate.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/tray_constants.h"
#include "base/utf_string_conversions.h"
#include "grit/ash_resources.h"
#include "grit/ash_strings.h"
#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/bubble/bubble_delegate.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

namespace {
const int kPaddingAroundBottomRow = 5;

// Create a label with the font size and color used in the network info bubble.
views::Label* CreateInfoBubbleLabel(const string16& text) {
  const SkColor text_color = SkColorSetARGB(127, 0, 0, 0);
  views::Label* label = new views::Label(text);
  label->SetFont(label->font().DeriveFont(-1));
  label->SetEnabledColor(text_color);
  return label;
}

// Create a row of labels for the network info bubble.
views::View* CreateInfoBubbleLine(const string16& text_label,
                                  const std::string& text_string) {
  views::View* view = new views::View;
  view->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kHorizontal, 0, 0, 1));
  view->AddChildView(CreateInfoBubbleLabel(text_label));
  view->AddChildView(CreateInfoBubbleLabel(UTF8ToUTF16(": ")));
  view->AddChildView(CreateInfoBubbleLabel(UTF8ToUTF16(text_string)));
  return view;
}

// A bubble that cannot be activated.
class NonActivatableSettingsBubble : public views::BubbleDelegateView {
 public:
  NonActivatableSettingsBubble(views::View* anchor, views::View* content)
      : views::BubbleDelegateView(anchor, views::BubbleBorder::TOP_RIGHT) {
    set_use_focusless(true);
    set_parent_window(ash::Shell::GetContainer(
        anchor->GetWidget()->GetNativeWindow()->GetRootWindow(),
        ash::internal::kShellWindowId_SettingBubbleContainer));
    SetLayoutManager(new views::FillLayout());
    AddChildView(content);
  }

  virtual ~NonActivatableSettingsBubble() {}

  virtual bool CanActivate() const OVERRIDE {
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NonActivatableSettingsBubble);
};

}  // namespace

namespace ash {
namespace internal {

namespace tray {

enum ColorTheme {
  LIGHT,
  DARK,
};

NetworkListDetailedViewBase::NetworkListDetailedViewBase(
    SystemTrayItem* owner,
    user::LoginStatus login,
    int header_string_id)
    : NetworkDetailedView(owner),
      login_(login),
      header_string_id_(header_string_id),
      info_icon_(NULL),
      settings_(NULL),
      proxy_settings_(NULL),
      info_bubble_(NULL) {
}

NetworkListDetailedViewBase::~NetworkListDetailedViewBase() {
  if (info_bubble_)
    info_bubble_->GetWidget()->CloseNow();
}

void NetworkListDetailedViewBase::Init() {
  CreateItems();
  Update();
  Shell::GetInstance()->tray_delegate()->RequestNetworkScan();
}

void NetworkListDetailedViewBase::Update() {
  UpdateAvailableNetworkList();
  UpdateHeaderButtons();
  UpdateNetworkEntries();
  UpdateNetworkExtra();

  Layout();
}

// Overridden from NetworkDetailedView:
NetworkDetailedView::DetailedViewType
    NetworkListDetailedViewBase::GetViewType() const {
  return NetworkDetailedView::LIST_VIEW;
}

void NetworkListDetailedViewBase::CreateItems() {
  RemoveAllChildViews(true);

  AppendNetworkEntries();
  AppendNetworkExtra();
  AppendHeaderEntry(header_string_id_);
  AppendHeaderButtons();
}

void NetworkListDetailedViewBase::AppendHeaderEntry(int header_string_id) {
  CreateSpecialRow(header_string_id, this);
}

void NetworkListDetailedViewBase::AppendNetworkExtra() {
  if (login_ == user::LOGGED_IN_LOCKED)
    return;

  views::BoxLayout* layout = new
    views::BoxLayout(views::BoxLayout::kHorizontal,
                     kPaddingAroundBottomRow,
                     kPaddingAroundBottomRow,
                     -1);
  layout->set_spread_blank_space(true);
  views::View* bottom_row = new View();
  bottom_row->SetLayoutManager(layout);

  AppendCustomButtonsToBottomRow(bottom_row);

  CreateSettingsEntry();
  DCHECK(settings_ || proxy_settings_);
  bottom_row->AddChildView(settings_ ? settings_ : proxy_settings_);

  AddChildView(bottom_row);

}

void NetworkListDetailedViewBase::AppendInfoButtonToHeader() {
  info_icon_ = new TrayPopupHeaderButton(
      this,
      IDR_AURA_UBER_TRAY_NETWORK_INFO,
      IDR_AURA_UBER_TRAY_NETWORK_INFO,
      IDR_AURA_UBER_TRAY_NETWORK_INFO_HOVER,
      IDR_AURA_UBER_TRAY_NETWORK_INFO_HOVER,
      IDS_ASH_STATUS_TRAY_NETWORK_INFO);
  info_icon_->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_NETWORK_INFO));
  footer()->AddButton(info_icon_);
}

void NetworkListDetailedViewBase::UpdateSettingButton() {
  if (proxy_settings_) {
    proxy_settings_->SetEnabled(
        Shell::GetInstance()->tray_delegate()->IsNetworkConnected());
  }
}

void NetworkListDetailedViewBase::UpdateAvailableNetworkList() {
  network_list_.clear();
  GetAvailableNetworkList(&network_list_);
}

void NetworkListDetailedViewBase::RefreshNetworkScrollWithUpdatedNetworkList() {
  network_map_.clear();
  std::set<std::string> new_service_paths;

  bool needs_relayout = false;
  views::View* highlighted_view = NULL;

  if (service_path_map_.empty())
    scroll_content()->RemoveAllChildViews(true);

  for (size_t i = 0; i < network_list_.size(); ++i) {
    std::map<std::string, HoverHighlightView*>::const_iterator it =
        service_path_map_.find(network_list_[i].service_path);
    HoverHighlightView* container = NULL;
    if (it == service_path_map_.end()) {
      // Create a new view.
      container = new HoverHighlightView(this);
      container->set_fixed_height(kTrayPopupItemHeight);
      container->AddIconAndLabel(network_list_[i].image,
          network_list_[i].description.empty() ?
              network_list_[i].name : network_list_[i].description,
          network_list_[i].highlight ?
              gfx::Font::BOLD : gfx::Font::NORMAL);
      scroll_content()->AddChildViewAt(container, i);
      container->set_border(views::Border::CreateEmptyBorder(0,
          kTrayPopupPaddingHorizontal, 0, 0));
      needs_relayout = true;
    } else {
      container = it->second;
      container->RemoveAllChildViews(true);
      container->AddIconAndLabel(network_list_[i].image,
          network_list_[i].description.empty() ?
              network_list_[i].name : network_list_[i].description,
          network_list_[i].highlight ? gfx::Font::BOLD : gfx::Font::NORMAL);
      container->Layout();
      container->SchedulePaint();

      // Reordering the view if necessary.
      views::View* child = scroll_content()->child_at(i);
      if (child != container) {
        scroll_content()->ReorderChildView(container, i);
        needs_relayout = true;
      }
    }

    if (network_list_[i].highlight)
      highlighted_view = container;
    network_map_[container] = network_list_[i].service_path;
    service_path_map_[network_list_[i].service_path] = container;
    new_service_paths.insert(network_list_[i].service_path);
  }

  std::set<std::string> remove_service_paths;
  for (std::map<std::string, HoverHighlightView*>::const_iterator it =
           service_path_map_.begin(); it != service_path_map_.end(); ++it) {
    if (new_service_paths.find(it->first) == new_service_paths.end()) {
      remove_service_paths.insert(it->first);
      scroll_content()->RemoveChildView(it->second);
      needs_relayout = true;
    }
  }

  for (std::set<std::string>::const_iterator remove_it =
           remove_service_paths.begin();
       remove_it != remove_service_paths.end(); ++remove_it) {
    service_path_map_.erase(*remove_it);
  }

  if (needs_relayout) {
    scroll_content()->SizeToPreferredSize();
    static_cast<views::View*>(scroller())->Layout();
    if (highlighted_view)
      scroll_content()->ScrollRectToVisible(highlighted_view->bounds());
  }
}

void NetworkListDetailedViewBase::ClearNetworkScrollWithEmptyNetworkList() {
  service_path_map_.clear();
  network_map_.clear();
  scroll_content()->RemoveAllChildViews(true);
}

void NetworkListDetailedViewBase::RefreshNetworkScrollWithUpdatedNetworkData() {
  if (network_list_.size() > 0 )
    RefreshNetworkScrollWithUpdatedNetworkList();
  else
    RefreshNetworkScrollWithEmptyNetworkList();
}

bool NetworkListDetailedViewBase::IsNetworkListEmpty() const {
  return network_list_.empty();
}

void NetworkListDetailedViewBase::ButtonPressed(views::Button* sender,
                                                const ui::Event& event) {
  if (sender == info_icon_) {
    ToggleInfoBubble();
    return;
  }

  // If the info bubble was visible, close it when some other item is clicked
  // on.
  ResetInfoBubble();
  ash::SystemTrayDelegate* delegate =
      ash::Shell::GetInstance()->tray_delegate();
  if (sender == settings_)
    delegate->ShowNetworkSettings();
  else if (sender == proxy_settings_)
    delegate->ChangeProxySettings();
  else
    CustomButtonPressed(sender, event);
}

void NetworkListDetailedViewBase::ClickedOn(views::View* sender) {
  ash::SystemTrayDelegate* delegate =
      ash::Shell::GetInstance()->tray_delegate();
  // If the info bubble was visible, close it when some other item is clicked
  // on.
  ResetInfoBubble();

  if (sender == footer()->content()) {
    owner()->system_tray()->ShowDefaultView(BUBBLE_USE_EXISTING);
    return;
  }

  if (login_ == user::LOGGED_IN_LOCKED)
    return;

  if (!CustomLinkClickedOn(sender)) {
    std::map<views::View*, std::string>::iterator find;
    find = network_map_.find(sender);
    if (find != network_map_.end()) {
      std::string network_id = find->second;
      delegate->ConnectToNetwork(network_id);
    }
  }
}

// Adds a settings entry when logged in, and an entry for changing proxy
// settings otherwise.
void NetworkListDetailedViewBase::CreateSettingsEntry() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  if (login_ != user::LOGGED_IN_NONE) {
    // Settings, only if logged in.
    settings_ = new TrayPopupLabelButton(this,
        rb.GetLocalizedString(IDS_ASH_STATUS_TRAY_NETWORK_SETTINGS));
  } else {
    proxy_settings_ = new TrayPopupLabelButton(this,
        rb.GetLocalizedString(IDS_ASH_STATUS_TRAY_NETWORK_PROXY_SETTINGS));
  }
}

views::View* NetworkListDetailedViewBase::CreateNetworkInfoView() {
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  std::string ip_address, ethernet_address, wifi_address;
  Shell::GetInstance()->tray_delegate()->GetNetworkAddresses(
      &ip_address, &ethernet_address, &wifi_address);

  views::View* container = new views::View;
  container->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kVertical, 0, 0, 1));
  container->set_border(views::Border::CreateEmptyBorder(0, 5, 0, 5));

  // GetNetworkAddresses returns empty strings if no information is available.
  if (!ip_address.empty()) {
    container->AddChildView(CreateInfoBubbleLine(bundle.GetLocalizedString(
        IDS_ASH_STATUS_TRAY_IP), ip_address));
  }
  if (!ethernet_address.empty()) {
    container->AddChildView(CreateInfoBubbleLine(bundle.GetLocalizedString(
        IDS_ASH_STATUS_TRAY_ETHERNET), ethernet_address));
  }
  if (!wifi_address.empty()) {
    container->AddChildView(CreateInfoBubbleLine(bundle.GetLocalizedString(
        IDS_ASH_STATUS_TRAY_WIFI), wifi_address));
  }

  // Avoid an empty bubble in the unlikely event that there is no network
  // information at all.
  if (!container->has_children()) {
    container->AddChildView(CreateInfoBubbleLabel(bundle.GetLocalizedString(
        IDS_ASH_STATUS_TRAY_NO_NETWORKS)));
  }

  return container;
}

void NetworkListDetailedViewBase::ToggleInfoBubble() {
  if (ResetInfoBubble())
    return;

  info_bubble_ = new NonActivatableSettingsBubble(
      info_icon_, CreateNetworkInfoView());
  views::BubbleDelegateView::CreateBubble(info_bubble_);
  info_bubble_->Show();
}

  // Returns whether an existing info-bubble was closed.
bool NetworkListDetailedViewBase::ResetInfoBubble() {
  if (!info_bubble_)
    return false;
  info_bubble_->GetWidget()->Close();
  info_bubble_ = NULL;
  return true;
}

}  // namespace tray

}  // namespace internal
}  // namespace ash
