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

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/notification_manager.hpp>

namespace brls
{

NotificationManager::NotificationManager()
{
    float width = Application::getStyle().getMetric("brls/notification/width");
    this->setWidth(width);
    this->setTranslationX(Application::ORIGINAL_WINDOW_WIDTH - width);
    this->setAxis(Axis::COLUMN);
}

void NotificationManager::notify(const std::string& text)
{
    brls::Logger::debug("Showing notification \"{}\"", text);

    auto style    = Application::getStyle();
    float timeout = style.getMetric("brls/animations/notification_timeout");
    float show    = style.getMetric("brls/animations/notification_show");
    float slide   = style.getMetric("brls/notification/slide");

    if (notificationFactory)
    {
        Box* view = notificationFactory(text);
        this->addView(view, 0);

        auto timer         = std::make_unique<Animatable>();
        Animatable* t      = timer.get();
        customTimers[view] = std::move(timer);

        t->reset(slide);
        t->addStep(0.0f, (int)show, EasingFunction::quadraticOut);
        t->addStep(0.0f, (int)timeout, EasingFunction::linear);
        t->addStep(slide, (int)show, EasingFunction::quadraticOut);

        t->setTickCallback([view, slide, t]()
            {
                float position = t->getValue();
                view->setTranslationX(position);
                view->setAlpha(1.0f - position / slide); });

        t->setEndCallback([this, view](bool)
            {
                this->removeView(view);
                customTimers.erase(view); });

        t->start();
    }
    else
    {
        auto* notification = new Notification(text);
        this->addView(notification, 0);

        notification->timeoutTimer.reset(slide);
        notification->timeoutTimer.addStep(0.0f, (int)show, EasingFunction::quadraticOut);
        notification->timeoutTimer.addStep(0.0f, (int)timeout, EasingFunction::linear);
        notification->timeoutTimer.addStep(slide, (int)show, EasingFunction::quadraticOut);

        notification->timeoutTimer.setTickCallback([notification, slide]()
            {
                float position = notification->timeoutTimer.getValue();
                notification->setTranslationX(position);
                notification->setAlpha(1.0f - position / slide); });

        notification->timeoutTimer.setEndCallback([this, notification](bool finished)
            { this->removeView(notification); });

        notification->timeoutTimer.start();
    }
}

void NotificationManager::setNotificationFactory(std::function<Box*(const std::string&)> factory)
{
    notificationFactory = std::move(factory);
}

NotificationManager::~NotificationManager()
{
    for (auto& [view, timer] : customTimers)
        timer->stop();
    customTimers.clear();

    std::vector<View*> views = this->getChildren();
    for (auto& view : views)
    {
        auto* notification = dynamic_cast<Notification*>(view);
        if (notification)
            notification->timeoutTimer.stop();
    }
}

Notification::Notification(const std::string& text)
{
    this->setBackground(ViewBackground::BACKDROP);
    auto style    = Application::getStyle();
    float padding = style.getMetric("brls/notification/padding");
    this->setPadding(padding);
    float width = style.getMetric("brls/notification/width");
    this->setWidth(width);
    this->label = new Label();
    this->label->setText(text);
    this->label->setTextColor(RGB(255, 255, 255));
    this->addView(label);
}

Notification::~Notification() = default;

}; // namespace brls
