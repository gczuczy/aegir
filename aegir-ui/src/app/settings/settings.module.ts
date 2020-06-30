import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { SettingsComponent } from './settings.component';

import { FormsModule, FormGroup, ReactiveFormsModule } from '@angular/forms';

@NgModule({
    imports: [
	CommonModule,
	FormsModule,
	ReactiveFormsModule,
    ],
    declarations: [SettingsComponent]
})
export class SettingsModule { }
