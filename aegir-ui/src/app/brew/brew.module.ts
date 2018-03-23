import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { UiSwitchModule } from 'ngx-ui-switch';
import { ChartsModule } from 'ng2-charts';

import { BrewComponent } from './brew.component';

@NgModule({
    imports: [
	CommonModule,
	ChartsModule,
	FormsModule,
	UiSwitchModule
    ],
    declarations: [BrewComponent]
})
export class BrewModule { }
