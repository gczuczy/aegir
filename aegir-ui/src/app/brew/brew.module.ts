import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';

import { ChartsModule } from 'ng2-charts';

import { BrewComponent } from './brew.component';

@NgModule({
    imports: [
	CommonModule,
	ChartsModule,
	FormsModule,
	MatSlideToggleModule
    ],
    declarations: [BrewComponent]
})
export class BrewModule { }
