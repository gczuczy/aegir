import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';

import { MaintenanceComponent } from './maintenance.component';

@NgModule({
    imports: [
	CommonModule,
	BrowserModule,
	FormsModule,
	MatSlideToggleModule
    ],
    declarations: [MaintenanceComponent],
    bootstrap: [MaintenanceComponent],
})
export class MaintenanceModule { }
