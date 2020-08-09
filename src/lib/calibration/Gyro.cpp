/****************************************************************************
 *
 *   Copyright (c) 2020 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include "Gyro.hpp"

#include <lib/calibration/Utilities.hpp>
#include <lib/parameters/param.h>

using namespace matrix;
using namespace sensors::calibration;
using namespace time_literals;

using math::radians;

namespace Sensors::Calibration
{

void Gyro::set_device_id(uint32_t device_id)
{
	if (_device_id != device_id) {
		_device_id = device_id;
		SensorCorrectionsUpdate(true);
		// TODO: ParametersUpdate();
	}
}

void Gyro::SensorCorrectionsUpdate(bool force)
{
	// check if the selected sensor has updated
	if (_sensor_correction_sub.updated() || force) {

		// valid device id required
		if (_device_id == 0) {
			return;
		}

		sensor_correction_s corrections;

		if (_sensor_correction_sub.copy(&corrections)) {
			// find sensor_corrections index
			for (int i = 0; i < MAX_SENSOR_COUNT; i++) {
				if (corrections.gyro_device_ids[i] == _device_id) {
					switch (i) {
					case 0:
						_thermal_offset = Vector3f{corrections.gyro_offset_0};
						return;
					case 1:
						_thermal_offset = Vector3f{corrections.gyro_offset_1};
						return;
					case 2:
						_thermal_offset = Vector3f{corrections.gyro_offset_2};
						return;
					}
				}
			}
		}

		// zero thermal offset if not found
		_thermal_offset.zero();
	}
}

void Gyro::ParametersUpdate()
{
	if (_device_id == 0) {
		return;
	}

	if (!_external) {
		_rotation = GetBoardRotation();

	} else {
		// TODO: per sensor external rotation
		_rotation.setIdentity();
	}


	int calibration_index = FindCalibrationIndex(SensorString(), _device_id);

	if (calibration_index >= 0) {
		int32_t priority_val = GetCalibrationParam(SensorString(), "PRIO", calibration_index);

		if (priority_val < 0 || priority_val > 100) {
			// reset to default
			PX4_ERR("%s %d invalid priorit %d, resetting to %d", SensorString(), calibration_index, priority_val, DEFAULT_PRIORITY);
			SetCalibrationParam(SensorString(), "PRIO", calibration_index, DEFAULT_PRIORITY);
		}

		_enabled = (priority_val > 0);

		_offset = GetCalibrationParamsVector3f(SensorString(), "OFF", calibration_index);

	} else {
		_enabled = true;
		_offset.zero();
	}
}

void Gyro::PrintStatus()
{
	PX4_INFO("%s %d EN: %d, offset: [%.4f %.4f %.4f]", SensorString(), _device_id, _enabled,
		 (double)_offset(0), (double)_offset(1), (double)_offset(2));

	if (_thermal_offset.norm() > 0.f) {
		PX4_INFO("%s %d temperature offset: [%.4f %.4f %.4f]", SensorString(), _device_id,
			 (double)_thermal_offset(0), (double)_thermal_offset(1), (double)_thermal_offset(2));
	}
}

} // namespace sensors
