import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { UiSwitchModule } from 'ngx-ui-switch';

import { MaintenanceComponent } from './maintenance.component';

@NgModule({
    imports: [
	CommonModule,
	BrowserModule,
	FormsModule,
	UiSwitchModule,
    ],
    declarations: [MaintenanceComponent],
    bootstrap: [MaintenanceComponent],
})
export class MaintenanceModule { }
