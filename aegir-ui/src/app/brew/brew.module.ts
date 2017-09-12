import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';

import { ChartsModule } from 'ng2-charts';

import { BrewComponent } from './brew.component';

@NgModule({
    imports: [
	CommonModule,
	ChartsModule
    ],
    declarations: [BrewComponent]
})
export class BrewModule { }
