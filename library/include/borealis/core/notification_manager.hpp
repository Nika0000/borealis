/*
Borealis, a Nintendo Switch UI Library
Copyright (C) 2019  natinusala
Copyright (C) 2024  xfangfang

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <borealis/core/animation.hpp>
#include <borealis/core/box.hpp>
#include <borealis/views/label.hpp>
#include <functional>
#include <memory>
#include <unordered_map>

// TODO: check in HOS that the animation duration + notification timeout are correct
namespace brls
{

class Notification : public Box
{
  public:
    explicit Notification(const std::string& text);
    ~Notification() override;

    Animatable timeoutTimer;

  private:
    Label* label;
};

class NotificationManager : public Box
{
  public:
    NotificationManager();
    ~NotificationManager() override;

    void notify(const std::string& text);

    /**
     * Sets a custom factory used to create notification views.
     * The factory receives the notification text and must return a Box* (which
     * NotificationManager will own, animate, and eventually destroy).
     * Set to nullptr to restore the default Notification view.
     */
    void setNotificationFactory(std::function<Box*(const std::string&)> factory);

  private:
    std::function<Box*(const std::string&)> notificationFactory;
    std::unordered_map<Box*, std::unique_ptr<Animatable>> customTimers;
};

}; // namespace brls
