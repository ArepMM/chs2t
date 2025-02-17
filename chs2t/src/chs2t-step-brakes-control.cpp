#include    "chs2t.h"

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void CHS2T::stepBrakesControl(double t, double dt)
{
    // Поездной кран машиниста
    brake_crane->setFLpressure(main_reservoir->getPressure());
    brake_crane->setBPpressure(brakepipe->getPressure());

    // Управляем краном, учитывая возможное наличие внешнего пульта
    if (control_signals.analogSignal[CS_BRAKE_CRANE].is_active)
    {
        int brake_crane_pos = static_cast<int>(control_signals.analogSignal[CS_BRAKE_CRANE].cur_value);
        brake_crane->setHandlePosition(brake_crane_pos);
    }
    else
    {
        brake_crane->setControl(keys);
    }

    brake_crane->step(t, dt);

    // Кран вспомогательного тормоза
    loco_crane->setFLpressure(main_reservoir->getPressure());
    loco_crane->setBCpressure(loco_crane_splitter->getInputPressure());
    loco_crane->setILpressure(0.0);

    // Управляем, с учетом возможного наличия пульта
    if (control_signals.analogSignal[CS_LOCO_CRANE].is_active)
    {
        double pos = control_signals.analogSignal[CS_LOCO_CRANE].cur_value;
        loco_crane->setHandlePosition(pos);
    }
    else
    {
        loco_crane->setControl(keys);
    }

    loco_crane->step(t, dt);

    // Рукоятка задатчика тормозного усилия
    handleEDT->setControl(keys, control_signals);
    handleEDT->step(t, dt);

    // Электропневматический вентиль экстренного торможения
    emergency_valve->setFLpressure(main_reservoir->getPressure());
    emergency_valve->setBPpressure(brakepipe->getPressure());
    emergency_valve->step(t, dt);

    // Электропневматический вентиль отпуска тормозов
    release_valve->setEDTcurrent(generator->getIa());
    release_valve->step(t, dt);

    // Управляющая камера воздухораспределителя (ложный ТЦ)
    brake_ref_res->setFlow(electro_air_dist->getBCflow());
    brake_ref_res->step(t, dt);

    // Разветвитель потока воздуха от локомотивного крана к тележкам
    loco_crane_splitter->setInputFlow(loco_crane->getBCflow());
    loco_crane_splitter->setPipePressure1(bc_switch_valve[TROLLEY_FWD]->getPressure1());
    loco_crane_splitter->setPipePressure2(bc_switch_valve[TROLLEY_BWD]->getPressure1());
    loco_crane_splitter->step(t, dt);

    // Скоростной клапан ДАКО
    double no_release = static_cast<double>(!release_valve->isPneumoBrakesRelease());
    dako->setAngularVelocity1(wheel_omega[0]);
    dako->setAngularVelocity6(wheel_omega[5]);
    dako->setFLpressure(main_reservoir->getPressure());
    dako->setBCpressure(bc_switch_valve[TROLLEY_BWD]->getPressure2());
    dako->setLocoCranePressure(max(loco_crane_splitter->getInputPressure(),
                                   emergency_valve->getAdditionalPressure()) );
    dako->setAirDistPressure(no_release * brake_ref_res->getPressure());
    dako->step(t, dt);

    // Повторительное реле давления №304
    // Подаёт в переключательный клапан передней тележки такое же давление,
    // какое установилось в клапане задней тележки от ДАКО и воздухораспределителя
    bc_pressure_relay->setFLpressure(main_reservoir->getPressure());
    bc_pressure_relay->setControlPressure(bc_switch_valve[TROLLEY_BWD]->getPressure2());
    bc_pressure_relay->setPipePressure(bc_switch_valve[TROLLEY_FWD]->getPressure2());
    bc_pressure_relay->step(t, dt);

    // Переключательные клапаны ЗПК потока в тормозные цилиндры
    // Первые входы клапана подключены к крану локомотивного тормоза
    // Вторые входы клапана подключены к ДАКО и воздухораспределителю
    // (у передней тележки через повторительное реле)
    // Выходы клапана подключены к ТЦ тележек
    bc_switch_valve[TROLLEY_FWD]->setInputFlow1(loco_crane_splitter->getPipeFlow1());
    bc_switch_valve[TROLLEY_FWD]->setInputFlow2(bc_pressure_relay->getPipeFlow());
    bc_switch_valve[TROLLEY_FWD]->setOutputPressure(brake_mech[TROLLEY_FWD]->getBCpressure());
    bc_switch_valve[TROLLEY_FWD]->step(t, dt);
    bc_switch_valve[TROLLEY_BWD]->setInputFlow1(loco_crane_splitter->getPipeFlow2());
    bc_switch_valve[TROLLEY_BWD]->setInputFlow2(dako->getBCflow());
    bc_switch_valve[TROLLEY_BWD]->setOutputPressure(brake_mech[TROLLEY_BWD]->getBCpressure());
    bc_switch_valve[TROLLEY_BWD]->step(t, dt);
}
