# 3D printing istruction

Some instruction for a better printing result.

3D File | Description / advices | Supports
--------|-----------------------|---------
Chassis.stl | Chassis where all the electronics/motors will be installed. | YES
Chassis_Armor_L.stl | Left chassis armor.|YES
Chassis_Armor_R.stl | Right chassis armor.| YES
Turret_Base.stl | Base where will lay the turret. It will be bolted to the chassis armor left + right | NO
Wheel_Armor_S_2PCS.stl <br> Wheel_Armor_T_2PCS.stl <br> Wheel_Armor_X_L.stl + Wheel_Armor_X_R.stl | Wheel Armor. You need 2x of this item. It will be bolted to the chassis armor left + right (one per side). <br>Print only one set, depending on your personal taste. | NO
Turret_Edelweiss.stl <br> Turret_MetalSlug.stl <br> Turret_PanzerIV.stl <br> Turret_Sherman.stl <br> Turret_TigerII.stl <br> Turret_X.stl | The tank turret. Print only one, depending on your personal taste. | YES
IR_Cover.stl | IR Receiver cover. It will be mounted on top of IR turret cover.| NO
IR_Turret_Cover.stl| IR turret cover. It keeps the IR Receiver and it will mounted on top of the turret. | NO
NodeMCU_Motor_Shield_battery.stl | Support for the NodeMCU motor shield and battery | NO
Track_32PCS.stl | Tank tracks. You need 32x of this item. In Cura software, you can add multiples istances of the same 3D object by simply right-clicking over the object and choosing "Multiply Selected Model" | NO
Wheel_Master_grub_2PCS.stl | Master Wheel. You need 2x of this item. It will be bolted to the N20 motor axle. <br> **ADVICE:** modify the support angle threshold to 85 degree in order to force the support only for the horizontal non touching surfaces. In Cura, use the "Support Overhang Angle" parameter. Remember to rollback the parameter value to the initial value after printing the wheels. | YES
Wheel_Slave_2PCS.stl | Slave wheel. You need 2x of this item. It will be fitted in the chassis. | NO
