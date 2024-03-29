// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_SYSTEM_TRAY_NOTIFIER_H_
#define ASH_SYSTEM_TRAY_SYSTEM_TRAY_NOTIFIER_H_

#include <string>
#include <vector>

#include "ash/ash_export.h"
#include "ash/system/audio/audio_observer.h"
#include "ash/system/bluetooth/bluetooth_observer.h"
#include "ash/system/brightness/brightness_observer.h"
#include "ash/system/chromeos/network/network_observer.h"
#include "ash/system/chromeos/network/sms_observer.h"
#include "ash/system/date/clock_observer.h"
#include "ash/system/drive/drive_observer.h"
#include "ash/system/ime/ime_observer.h"
#include "ash/system/locale/locale_observer.h"
#include "ash/system/logout_button/logout_button_observer.h"
#include "ash/system/power/power_status_observer.h"
#include "ash/system/tray_accessibility.h"
#include "ash/system/tray_caps_lock.h"
#include "ash/system/user/update_observer.h"
#include "ash/system/user/user_observer.h"
#include "base/observer_list.h"

#if defined(OS_CHROMEOS)
#include "ash/system/chromeos/network/network_observer.h"
#include "ash/system/chromeos/network/sms_observer.h"
#endif

namespace ash {

class ASH_EXPORT SystemTrayNotifier {
public:
  SystemTrayNotifier();
  ~SystemTrayNotifier();

  void AddAccessibilityObserver(AccessibilityObserver* observer);
  void AddAudioObserver(AudioObserver* observer);
  void AddBluetoothObserver(BluetoothObserver* observer);
  void AddBrightnessObserver(BrightnessObserver* observer);
  void AddCapsLockObserver(CapsLockObserver* observer);
  void AddClockObserver(ClockObserver* observer);
  void AddDriveObserver(DriveObserver* observer);
  void AddIMEObserver(IMEObserver* observer);
  void AddLocaleObserver(LocaleObserver* observer);
  void AddLogoutButtonObserver(LogoutButtonObserver* observer);
  void AddPowerStatusObserver(PowerStatusObserver* observer);
  void AddUpdateObserver(UpdateObserver* observer);
  void AddUserObserver(UserObserver* observer);
#if defined(OS_CHROMEOS)
  void AddNetworkObserver(NetworkObserver* observer);
  void AddVpnObserver(NetworkObserver* observer);
  void AddSmsObserver(SmsObserver* observer);
#endif

  void NotifyAccessibilityModeChanged(bool spoken_feedback_enabled);
  void NotifyVolumeChanged(float level);
  void NotifyMuteToggled();
  void NotifyRefreshBluetooth();
  void NotifyBluetoothDiscoveringChanged();
  void NotifyBrightnessChanged(double level, bool user_initialted);
  void NotifyCapsLockChanged(bool enabled, bool search_mapped_to_caps_lock);
  void NotifyRefreshClock();
  void NotifyDateFormatChanged();
  void NotifyRefreshDrive(DriveOperationStatusList& list);
  void NotifyRefreshIME(bool show_message);
  void NotifyShowLoginButtonChanged(bool show_login_button);
  void NotifyLocaleChanged(LocaleObserver::Delegate* delegate,
                           const std::string& cur_locale,
                           const std::string& from_locale,
                           const std::string& to_locale);
  void NotifyPowerStatusChanged(const PowerSupplyStatus& power_status);
  void NotifyUpdateRecommended(UpdateObserver::UpdateSeverity severity);
  void NotifyUserUpdate();
#if defined(OS_CHROMEOS)
  void NotifyRefreshNetwork(const NetworkIconInfo &info);
  void NotifySetNetworkMessage(NetworkTrayDelegate* delegate,
                               NetworkObserver::MessageType message_type,
                               NetworkObserver::NetworkType network_type,
                               const string16& title,
                               const string16& message,
                               const std::vector<string16>& links);
  void NotifyClearNetworkMessage(NetworkObserver::MessageType message_type);
  void NotifyVpnRefreshNetwork(const NetworkIconInfo &info);
  void NotifyWillToggleWifi();
  void NotifyAddSmsMessage(const base::DictionaryValue& message);
#endif

 private:
  ObserverList<AccessibilityObserver> accessibility_observers_;
  ObserverList<AudioObserver> audio_observers_;
  ObserverList<BluetoothObserver> bluetooth_observers_;
  ObserverList<BrightnessObserver> brightness_observers_;
  ObserverList<CapsLockObserver> caps_lock_observers_;
  ObserverList<ClockObserver> clock_observers_;
  ObserverList<DriveObserver> drive_observers_;
  ObserverList<IMEObserver> ime_observers_;
  ObserverList<LocaleObserver> locale_observers_;
  ObserverList<LogoutButtonObserver> logout_button_observers_;
  ObserverList<PowerStatusObserver> power_status_observers_;
  ObserverList<UpdateObserver> update_observers_;
  ObserverList<UserObserver> user_observers_;
#if defined(OS_CHROMEOS)
  ObserverList<NetworkObserver> network_observers_;
  ObserverList<NetworkObserver> vpn_observers_;
  ObserverList<SmsObserver> sms_observers_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SystemTrayNotifier);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_SYSTEM_TRAY_NOTIFIER_H_
