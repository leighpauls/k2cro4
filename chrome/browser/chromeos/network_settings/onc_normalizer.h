// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NETWORK_SETTINGS_ONC_NORMALIZER_H_
#define CHROME_BROWSER_CHROMEOS_NETWORK_SETTINGS_ONC_NORMALIZER_H_

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chromeos/network_settings/onc_mapper.h"

namespace chromeos {
namespace onc {

struct OncValueSignature;

class Normalizer : public Mapper {
 public:
  explicit Normalizer(bool remove_recommended_fields);
  virtual ~Normalizer();

  // Removes all fields that are ignored/irrelevant because of the value of
  // other fields. E.g. the "WiFi" field is irrelevant if the configurations
  // type is "Ethernet". If |remove_recommended_fields| is true, kRecommended
  // arrays are removed (the array itself and not the field names listed
  // there). |object_signature| must point to one of the signatures in
  // |onc_signature.h|.
  scoped_ptr<base::DictionaryValue> NormalizeObject(
      const OncValueSignature* object_signature,
      const base::DictionaryValue& onc_object);

 private:
  // Dispatch to the right normalization function according to |signature|.
  virtual scoped_ptr<base::DictionaryValue> MapObject(
      const OncValueSignature& signature,
      const base::DictionaryValue& onc_object) OVERRIDE;

  void NormalizeIPsec(base::DictionaryValue* ipsec);
  void NormalizeVPN(base::DictionaryValue* vpn);
  void NormalizeNetworkConfiguration(base::DictionaryValue* network);

  const bool remove_recommended_fields_;

  DISALLOW_COPY_AND_ASSIGN(Normalizer);
};

}  // namespace onc
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NETWORK_SETTINGS_ONC_NORMALIZER_H_
